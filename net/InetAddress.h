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

	// ����ֻ�ǽ���ָ�����ͽ���ת������ʹ�� addr6_ �滻Ϊ adr_ ���ɿ���
	// ������ü� toIp() AND toIpPort()
	const struct sockaddr* getSockaddr() const { return sockets::sockaddr_cast(&addr6_); }
	void setSocketAddrInet6(const struct sockaddr_in6 addr6) { addr6_ = addr6; }

	// ���� ipv4 ��ַ�������ֽ���
	uint32_t ipv4NetEndian() const;
	// ���� ipv4 �˿ڵ������ֽ���
	uint16_t portNetEndian() const { return addr_.sin_port; }

	// ���� ipv6 ��Ÿ�������ֽ��� - ����ϲ�����������ټ���
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
* ���˸��ܣ�muduo ������ InetAddress ��ؽӿ���Щ̫�����ˣ��ҷ�����д����ֲ�������ӿڵ��ô�
*			���Ҹ�����˵������ҵĿ��ܾ���ΪʲôһЩ�ӿ�ֻ�ṩ IPv4 �����ṩ IPv6 ��ģ�
*			��������һЩ������Ľӿڣ�Ϊʲô ipv4NetEndian() �ṩ assert ���Լ�� AF_INET���� portNetEndian() ȷ���ṩ�أ�����
*			����Ǵ�д��ħ�ҡ���
* PS: ��������湹���ϲ����ʱ������������̫�����ˣ������ģ�齫���� Misaka ��
*/

#endif // !MISAKA_INETADDRESS_H

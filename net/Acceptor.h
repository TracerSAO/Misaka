#ifndef MISAKA_ACCEPTOR_H
#define MISAKA_ACCEPTOR_H

#include "../base/noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>
#include <memory>

namespace Misaka
{
namespace net
{

class EventLoop;

class Acceptor : noncopyable
{
public:
	typedef std::function<void(int sockfd, const InetAddress&)> NewConnectionCallback;

public:
	Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
	~Acceptor();

	void listen();

	void setNewConnectionCallback(const NewConnectionCallback&);

	bool listening() const { return listening_; }

private:
	void handleRead();

private:
	EventLoop* loop_;
	Socket acceptSocket_;
	Channel acceptChannel_;
	NewConnectionCallback newConnectionCallback_;
	bool listening_;
	int idleFd_;	// 备用一个 idle(空闲) 文件描述符 -> 应对文件描述符不够的情况
};

}	// namespace net
}	// namespace Misaka

#endif // !MISAKA_ACCEPTOR_H

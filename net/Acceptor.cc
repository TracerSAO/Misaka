#include "Acceptor.h"
#include "EventLoop.h"
#include "SocketsOps.h"
#include "InetAddress.h"

#include <fcntl.h>

using namespace Misaka;
using namespace net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport) :
	loop_(loop),
	acceptSocket_(sockets::createNonblockingOrDie(AF_INET)),
	acceptChannel_(loop, acceptSocket_.fd()),
	listening_(false),
	idleFd_(open("/dev/null", O_RDONLY | O_CLOEXEC))
{
	assert(0 <= idleFd_);
	acceptSocket_.setReuseAddr(true);
	acceptSocket_.setReusePort(reuseport);
	acceptSocket_.bindAddress(listenAddr);
	acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
	acceptChannel_.disableAll();
	acceptChannel_.remove();
	::close(idleFd_);
}

void Acceptor::listen()
{
	loop_->assertInLoopThread();
	listening_ = true;
	acceptSocket_.listen();
	acceptChannel_.enableReading();
}

void Acceptor::setNewConnectionCallback(const NewConnectionCallback& newConnectionCallback)
{
	newConnectionCallback_ = newConnectionCallback;
}

void Acceptor::handleRead()
{
	loop_->assertInLoopThread();	// 确保线程安全
	InetAddress addr;
	int connfd = acceptSocket_.accept(&addr);
	if (0 <= connfd)
	{
		if (newConnectionCallback_)
			newConnectionCallback_(connfd, addr);
		else
			sockets::close(connfd);
	}
	else
	{
		// FIXME: 使用 LOG 替换
		assert(0 > connfd && EMFILE == errno);
		::close(idleFd_);
		idleFd_ = ::accept(acceptSocket_.fd(), nullptr, nullptr);
		::close(idleFd_);
		idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
	}
}
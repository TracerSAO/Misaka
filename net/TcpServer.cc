#include "../base/Timestamp.h"
#include "../base/Logging.h"
#include "TcpServer.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

#include "Socket.h"
#include "InetAddress.h"
#include "SocketsOps.h"
#include "Acceptor.h"
#include "TcpConnection.h"

using namespace Misaka;
using namespace net;

TcpServer::TcpServer(EventLoop* loop,
						const string& nameArg,
						const InetAddress& listenAddr,
						Option option) :
	loop_(loop),
	ipPort_(listenAddr.toIpPort()),
	name_(nameArg),
	acceptor(new Acceptor(loop_, listenAddr,
						  Option::kReuseport == option ? true : false)),
	threadpool_(new EventLoopThreadPool(loop_, name_)),
	connectionCallback_(Misaka::net::defaultConnectionCallback),
	messageCallback_(Misaka::net::defaultMessageCallback),
	nextConnId_(1)
{
	assert(loop_ && acceptor && threadpool_);
	acceptor->setNewConnectionCallback(
		std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
	loop_->assertInLoopThread();	// 此处为强校验, 因为 TcpServer 从始至终都只会存在于一个 thread
	for (auto& item : connections_)
	{
		TcpConnectionPtr conn(item.second);
		item.second.reset();
		conn->connectionDestroyed();
		EventLoop* ioLoop = conn->getLoop();
		assert(nullptr != ioLoop);
		ioLoop->runInLoop(std::bind(
			&TcpConnection::connectionDestroyed, conn));
	}
}

void TcpServer::setThreadNum(int numThreads)
{
	loop_->assertInLoopThread();
	assert(!threadpool_->started());	// setThreadNum() 必须在 start() 之前调用
	threadpool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
	if (start_.getAndSet(1) == 0)
	{
		threadpool_->start(threadInitCallback_);
		assert(!acceptor->listening());
		loop_->runInLoop(
			std::bind(&Acceptor::listen, get_pointer(acceptor)) );
	}
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
	// 获取 new TcpConnection 的名字
	char buf[64];
	snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
	nextConnId_++;
	string connName = name_ + buf;

	// 打日志
	LOG_INFO << "TcpServer::newConnection [" << name_
			 << "] - new connection [" << connName
			 << "] from " << peerAddr.toIpPort();

	// 获取 new connection 所分配的 EventLoop
	EventLoop* ioLoop = threadpool_->getNextLoop();

	// 创建 new TcpConnection
	InetAddress localAddr(sockets::getLocalAddr(sockfd));
	TcpConnectionPtr conn(new TcpConnection(ioLoop,
											connName,
											sockfd,
											localAddr,
											peerAddr));
	
	// 记录在案
	connections_[connName] = conn;
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writecompleteCallback_);
	conn->setCloseCallback(std::bind(
		&TcpServer::removeConnection, this, _1));
	ioLoop->runInLoop(std::bind(
		&TcpConnection::connectionEstablished, conn));

	// 向 TcpConnection 归属的 EventLoop 事件监控单元注册这个 TcpConnection
	// TcpConnection 的监听事件同 Acceptor 一样，都是只有在触发条件的情况下才会开启监听，
	// 不同于 TimerQueue, TimerQueue 是只要 obj 诞生，默认开启监听
	// 以上所陈述的不同，是基于不同的事件处理来分别的
	//conn->connectionEstablished();
	
	// 这种写法，是用于 multi-TcpServe
	// 且这种写法，也是实现 TcpConnection 向不同 EventLoop 分配的关键
	// 只有跨线程，才需要使用 runInLoop() 这里就是为了将 本属于 main-Loop 的东西发送到 other-Loop
	//loop_->runInLoop(
	//	std::bind(&TcpConnection::connectionEstablished, get_pointer(conn)) );
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
	loop_->runInLoop(std::bind(
		&TcpServer::removeConnectionInLoop, this, conn) );
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
	loop_->assertInLoopThread();
	EventLoop* ioLoop = conn->getLoop();
	assert(nullptr != ioLoop);

	LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
			 << "] - connection " << conn->name();

	size_t n = connections_.erase(conn->name());
	assert(1 == n);
	ioLoop->runInLoop(std::bind(
		&TcpConnection::connectionDestroyed, conn));
}

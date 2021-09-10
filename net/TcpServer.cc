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
	loop_->assertInLoopThread();	// �˴�ΪǿУ��, ��Ϊ TcpServer ��ʼ���ն�ֻ�������һ�� thread
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
	assert(!threadpool_->started());	// setThreadNum() ������ start() ֮ǰ����
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
	// ��ȡ new TcpConnection ������
	char buf[64];
	snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
	nextConnId_++;
	string connName = name_ + buf;

	// ����־
	LOG_INFO << "TcpServer::newConnection [" << name_
			 << "] - new connection [" << connName
			 << "] from " << peerAddr.toIpPort();

	// ��ȡ new connection ������� EventLoop
	EventLoop* ioLoop = threadpool_->getNextLoop();

	// ���� new TcpConnection
	InetAddress localAddr(sockets::getLocalAddr(sockfd));
	TcpConnectionPtr conn(new TcpConnection(ioLoop,
											connName,
											sockfd,
											localAddr,
											peerAddr));
	
	// ��¼�ڰ�
	connections_[connName] = conn;
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writecompleteCallback_);
	conn->setCloseCallback(std::bind(
		&TcpServer::removeConnection, this, _1));
	ioLoop->runInLoop(std::bind(
		&TcpConnection::connectionEstablished, conn));

	// �� TcpConnection ������ EventLoop �¼���ص�Ԫע����� TcpConnection
	// TcpConnection �ļ����¼�ͬ Acceptor һ��������ֻ���ڴ�������������²ŻῪ��������
	// ��ͬ�� TimerQueue, TimerQueue ��ֻҪ obj ������Ĭ�Ͽ�������
	// �����������Ĳ�ͬ���ǻ��ڲ�ͬ���¼��������ֱ��
	//conn->connectionEstablished();
	
	// ����д���������� multi-TcpServe
	// ������д����Ҳ��ʵ�� TcpConnection ��ͬ EventLoop ����Ĺؼ�
	// ֻ�п��̣߳�����Ҫʹ�� runInLoop() �������Ϊ�˽� ������ main-Loop �Ķ������͵� other-Loop
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

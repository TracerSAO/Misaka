#ifndef MISAKA_TCPSERVER_H
#define MISAKA_TCPSERVER_H

#include "../base/noncopyable.h"
#include "../base/Atomic.h"
#include "callbacks.h"
#include "Channel.h"

#include "TcpConnection.h"

#include <map>

namespace Misaka
{
namespace net
{
	
class EventLoop;
class Acceptor;
class EventLoopThreadPool;

class TcpServer : noncopyable
{
public:
	typedef std::function<void (EventLoop*)> ThreadInitCallback;
	enum class Option
	{
		kNoReuseport,
		kReuseport
	};
public:
	TcpServer(EventLoop* loop,
				const string& nameArg,
				const InetAddress& listenAddr,
				Option option = Option::kNoReuseport);
	~TcpServer();

	const string& isPort() const { return ipPort_; }
	const string& name() const { return name_; }
	EventLoop* getLoop() const { return loop_; }

	void start();

	// ����ӿڣ�user ������ TcpServer ����ǰ�趨�� ThreadNum
	void setThreadNum(int numThreads);
	// ����ӿڣ�Ϊ user �ṩһ�������� multi-thread �����󣬿���ִ�еĶ�������ô�ÿ��û��Լ�
	// eg: �����趨һ�� ��ÿ�� 10 ���ӡһ�� 'bilibili~ 2233' �Ķ�ʱ������
	void setThreadInitCallback(const ThreadInitCallback& cb)
	{ threadInitCallback_ = cb; }

	void setConnectionCallback(ConnectionCallback cb)
	{ connectionCallback_ = cb; }
	void setMessageCallback(MessageCallback cb)
	{ messageCallback_ = cb; }
	void setWriteCompleteCallback(WriteCompleteCallback cb)
	{ writecompleteCallback_ = cb; }

private:
	void newConnection(int sockfd, const InetAddress& addr);
	void removeConnection(const TcpConnectionPtr&);
	void removeConnectionInLoop(const TcpConnectionPtr&);

private:
	using ConnectionMap = std::map<string, TcpConnectionPtr>;

	EventLoop* loop_;
	const string ipPort_;
	const string name_;
	
	std::unique_ptr<Acceptor> acceptor;
	std::shared_ptr<EventLoopThreadPool> threadpool_;
	ConnectionCallback connectionCallback_;
	MessageCallback messageCallback_;
	WriteCompleteCallback writecompleteCallback_;
	ThreadInitCallback threadInitCallback_;
	AtomicInt32 start_;

	int nextConnId_;
	ConnectionMap connections_;
};

}	// namespace net
}	// namespace Misaka

#endif // !MISAKA_TCPSERVER_H

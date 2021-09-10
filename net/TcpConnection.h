#ifndef MISAKA_TCPCONNECTION_H
#define MISAKA_TCPCONNECTION_H

#include "../base/Timestamp.h"
#include "../base/noncopyable.h"
#include "callbacks.h"
#include "Buffer.h"
#include "InetAddress.h"

#include <memory>
#include <any>

//defined in <netinet/tcp.h>
struct tcp_info;

namespace Misaka
{
namespace net
{

class EventLoop;
class Channel;
class Socket;

class TcpConnection : noncopyable,
						public std::enable_shared_from_this<TcpConnection>
{
public:
	TcpConnection(EventLoop* loop, 
					const string& connName,
					int sockfd,
					const InetAddress& localaddr,
					const InetAddress& peeraddr);
	~TcpConnection();

	const string& name() const { return connName_; }
	EventLoop* getLoop() { return loop_; }
	const InetAddress& localAddress() const { return localaddr_; }
	const InetAddress& peerAddress() const { return peeraddr_; }
	bool connected() const { return STATE::connected == state_; }
	bool disconnected() const { return STATE::disconnected == state_; }
	bool getTcpInfo(struct tcp_info*) const;
	string getTcpInfoString() const;

	void send(const void*, int);

	void send(const StringPiece&);
	
	void send(Buffer*);

	void shutdown();

	void setTcpNotDelay(bool on);

	// forceClose-interface 并不是提供给 TcpConnection 自己使用的，
	// 而是给外部用户使用，向用户提供可以强制 close connection 的功能力，
	// -> 很自然的，user 能在哪里删除 connection 呢？肯定不是在 TcpConnection 自己的线程之中
	// -> 所以，loop_->runInLoop() 就不会有 inLoop() 这一说
	// -> 所以，直接绕过 loop_->runInLoop()，使用 loop_->queueInLoop()
	void forceClose();
	
	void forceCloseWithDelay(double seconds);

	void setContext(const std::any& context)
	{ context_ = context; }

	const std::any& getContext() const
	{ return context_; }

	std::any* getMutableContext()
	{ return &context_; }

	void setConnectionCallback(const ConnectionCallback& cb)
	{ connectionCallback_ = cb; }

	void setMessageCallback(const MessageCallback& cb)
	{ messageCallback_ = cb; }

	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{ writecompleteCallback_ = cb; }

	void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
	{ highwaterCallback_ = cb; highWaterMark_ = highWaterMark; }

	Buffer* inputBuffer()
	{ return &inputBuf_; }

	Buffer* outputBuffer()
	{ return &outputBuf_; }

	// internal use only -> support for TcpServer
	void setCloseCallback(const CloseCallback& cb)
	{ closeCallback_ = cb; }

	// thread unsafe, but used in Loop
	void connectionEstablished();	// will be called only once
	// thread unsafe, but used in Loop
	void connectionDestroyed();		// will be called only once

private:
	enum class STATE
	{
		connecting,
		connected,
		disconnecting,
		disconnected
	};

	void handleRead(Timestamp);
	void handleWrite();
	void handleClose();
	void handleError();

	void setState(STATE state) { state_ = state; }
	void sendInLoop(const StringPiece&);
	void sendInLoop(const void*, size_t);
	void shutdownInLoop();
	void forceCloseInLoop();
	const char* stateToString() const;

private:
	EventLoop* loop_;
	STATE state_;
	const string connName_;
	std::unique_ptr<Socket> socket_;
	std::unique_ptr<Channel> channel_;
	const InetAddress localaddr_;
	const InetAddress peeraddr_;

	ConnectionCallback connectionCallback_;
	MessageCallback messageCallback_;
	WriteCompleteCallback writecompleteCallback_;
	HighWaterMarkCallback highwaterCallback_;
	CloseCallback closeCallback_;

	size_t highWaterMark_;	// 缓存最高警戒线
	Buffer inputBuf_;
	Buffer outputBuf_;

	std::any context_;
};

}	// namespace net
}	// namespace Misaka

#endif // !MISAKA_TCPCONNECTION_H

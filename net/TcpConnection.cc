#include "../base/Logging.h"
#include "../base/WeakCallback.h"
#include "EventLoop.h"
#include "Channel.h"
#include "TimerId.h"

#include "InetAddress.h"
#include "Socket.h"
#include "SocketsOps.h"
#include "TcpConnection.h"

using namespace Misaka;
using namespace net;

void Misaka::net::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
	// do-noting
}

void Misaka::net::defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* meg, Timestamp receiveTime)
{
	meg->retrieveAll();	// 清空自己刚收集到的数据
}

TcpConnection::TcpConnection(EventLoop* loop,
								const string& connName,
								int sockfd,
								const InetAddress& localaddr,
								const InetAddress& peeraddr) :
	loop_(CHECKNOTNULL(loop)),
	state_(STATE::connecting),
	connName_(connName),
	socket_(new Socket(sockfd)),
	channel_(new Channel(loop, sockfd)),
	localaddr_(localaddr),
	peeraddr_(peeraddr),
	highWaterMark_(64*1024*1024)
{
	assert(socket_ && channel_);
	channel_->setReadCallback(
		std::bind(&TcpConnection::handleRead,  this, _1));
	channel_->setWriteCallback(
		std::bind(&TcpConnection::handleWrite, this));
	channel_->setCloseCallback(
		std::bind(&TcpConnection::handleClose, this));
	channel_->setErrorCallback(
		std::bind(&TcpConnection::handleError, this));

	LOG_DEBUG << "TcpConnection::TcpConnection() - [" << connName_ << "] at " << this
			  << " fd=" << socket_->fd();

	// 默认开启 TCP-KeepAlive 选项
	socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
	LOG_DEBUG << "TcpConnection::~TcpConnection() - [" << connName_ << "] at " << this
			  << "] fd=" << socket_->fd()
			  << " state= " << stateToString();
	assert(STATE::disconnected == state_);
}

const char* TcpConnection::stateToString() const
{
	switch (state_)
	{
	case STATE::connecting:
		return "connecting";
	case STATE::connected:
		return "connected";
	case STATE::disconnecting:
		return "disconnecting";
	case STATE::disconnected:
		return "disconnected";
	default:
		return "unknow-state!";
	}
}

bool TcpConnection::getTcpInfo(struct tcp_info* info) const
{
	return socket_->getTcpInfo(info);
}

string TcpConnection::getTcpInfoString() const
{
	char buf[1024];
	buf[0] = '\0';
	socket_->getTcpInfoString(buf, sizeof buf);
	return buf;
}

void TcpConnection::setTcpNotDelay(bool on)
{
	socket_->setTcpNoDelay(on);
}

void TcpConnection::send(const void* meg, int len)
{
	// loop_->runInLoop(std::bind(
	// 	&TcpConnection::sendInLoop, this, meg, len));	// error -> 原因暂时不明，解决方案为: 显示声明自己想要绑定的函数

	send(StringPiece(static_cast<const char*>(meg), len));
}

void TcpConnection::send(const StringPiece& meg)
{
	if (STATE::connected == state_)
	{
		if (loop_->isInLoopThread())
		{
			sendInLoop(meg);
		}
		else
		{
			void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
			loop_->runInLoop(std::bind(fp, this, std::move(meg.as_string()) ));
			// 使用 as_string() 是因为 send() 支持线程安全，所以，不能传递引用或指针，
			// 因为无法保证数据源是否发生改变
		}
	}
}

void TcpConnection::send(Buffer* meg)
{
	if (STATE::connected == state_)
	{
		if (loop_->isInLoopThread())
		{
			sendInLoop(meg->toStringPiece());
		}
		else
		{
			void (TcpConnection:: * fd)(const StringPiece & message) = &TcpConnection::sendInLoop;
			loop_->runInLoop(std::bind(fd, this, std::move(meg->toStringPiece().as_string()) ));
		}
	}
}

void TcpConnection::sendInLoop(const StringPiece& meg)
{
	sendInLoop(meg.data(), meg.size());
}

void TcpConnection::sendInLoop(const void* meg, size_t len)
{
	loop_->assertInLoopThread();
	ssize_t nwrote = 0;
	bool faultError = false;
	size_t remain = len;
	if (STATE::disconnected == state_)
	{
		LOG_WARN << "TcpConnection::sendInLoop() disconnected - give up writing";
		return;
	}
	if (!channel_->isWriting() && 0 == outputBuf_.readableBytes())
	{
		nwrote = sockets::write(socket_->fd(), meg, len);
		if (0 <= nwrote)
		{
			remain -= nwrote;
			if (0 == remain && writecompleteCallback_)
			{	// 当数据全部发送完毕后，触发写完毕事件 -> 发给 Loop 去处理 -> 确保高优先级的事件先处理 (read 事件 OR write 事件 OR ...)
				loop_->queueInLoop(std::bind(writecompleteCallback_, shared_from_this()));
			}
		}
		else
		{
			nwrote = 0;
			if (EWOULDBLOCK == errno)
			{
				LOG_SYSERR << "TcpConnection::sendInLoop()";;
				if (EPIPE == errno || ECONNRESET == errno)
					faultError = true;	// 这种异常，属于对端连接出现问题，ta 无法接受数据，我们也不应该再给 ta send
			}
		}
	}

	assert(remain <= len);
	if (!faultError && 0 < remain)
	{
		size_t old_len = outputBuf_.readableBytes();
		if (old_len + remain > highWaterMark_ &&
			old_len < highWaterMark_ &&
			highwaterCallback_)
		{	// 强制性发给 Loop 去处理 -> 貌似算作是“利用 Poller 来均摊事件处理复杂度” -> 将更多的事情转移给下次处理 -> 优先处理更紧急的响应任务
			loop_->queueInLoop(std::bind(highwaterCallback_, shared_from_this(), old_len + remain));
		}
		outputBuf_.append(static_cast<const char*>(meg) + nwrote, remain);
		if (!channel_->isWriting())
		{
			channel_->enableWriting();
		}
	}
}

void TcpConnection::shutdown()
{
	if (STATE::connected == state_)
	{
		setState(STATE::disconnecting);
		void (TcpConnection:: * fd)() = &TcpConnection::shutdownInLoop;
		loop_->runInLoop(std::bind(fd, this));
	}
}

void TcpConnection::shutdownInLoop()
{
	loop_->assertInLoopThread();
	if (!channel_->isWriting())
	{
		sockets::shutdownWrite(socket_->fd());
	}
}

void TcpConnection::forceClose()
{
	if (STATE::connected == state_ || STATE::disconnecting == state_)
	{
		setState(STATE::disconnecting);
		loop_->runInLoop(std::bind(
			&TcpConnection::forceCloseInLoop, shared_from_this() ));
	}
}

void TcpConnection::forceCloseWithDelay(double seconds)
{
	if (STATE::connected == state_ || STATE::disconnecting == state_)
	{
		setState(STATE::disconnecting);
		//loop_->runAfter(seconds, std::bind(
		//	&TcpConnection::forceClose, shared_from_this() ));	// 不够安全，一点指针悬空(shared_ptr null) 程序直接崩溃
		loop_->runAfter(seconds,
			makeWeakCallback(shared_from_this(), &TcpConnection::forceClose));
		// avoid race condition which from "forceCloseInLoop"
		// 使用 WeakCallback() 的目的：利用弱引用，检测 TcpConnection 对象是否还存在，但不能使用 shared_ptr<> 否则会掩藏 ta 的生命周期
		// 你问怎么延长的?
		// 你拿抱着一份引用，在 Poller 里等着调用，你绝对 TCPConnection 在这段时间里能被 delete 掉吗? :)
	}
	// 假如我先设定了 42 seconds 后自动断开连接
	// 42 seconds 之内，我突然绝对不妥，需要提前关闭连接，
	// 结果提前也没提前多少 -> 提前关闭和自动关闭的时间被同一次的 Poller 捕获
	// Res: 先执行提前关闭 success，后执行自动关闭，发生 assert() 断言错误
}

void TcpConnection::forceCloseInLoop()
{
	loop_->assertInLoopThread();
	if (STATE::connected == state_ || STATE::disconnecting == state_)
	{
		handleClose();
	}
}

void TcpConnection::connectionEstablished()
{
	loop_->assertInLoopThread();
	assert(STATE::connecting == state_);
	setState(STATE::connected);

	// 利用 Channel_->tie_ [std::weak_ptr<>] 来延长 TcpConnection 的生命
	// 确保，Tcpconnection 可以挺到 Channel::handleEvent() 这个函数调用完毕
	// 不然就是出现：【欧皇：core-dump】 【非酋：assert xxxx failed ????】
	channel_->tie(shared_from_this());
	channel_->enableReading();

	connectionCallback_(shared_from_this());
}

void TcpConnection::connectionDestroyed()
{
	loop_->assertInLoopThread();
	// 这一块儿，我与 muduo 的意见不一致
	// 我认为有必要，同时判断 STATE::disconnecting 状态，
	// 分歧的原因主要在语音 connectionDestroyed() 对外的语义，是给谁用
	if (STATE::connected == state_ || STATE::disconnecting == state_)
	{
		setState(STATE::disconnected);
		channel_->disableAll();

		connectionCallback_(shared_from_this());
	}
	LOG_INFO << "TcpConnection::connectionDestroyed()";
	channel_->remove();

	// PS: 到这里，就是到了 TcpConnection 生命的尽头，除非 usr 持有当前的 connection
	// PS: 到这里，如果是 handleRead() 触发 connectionClose，则还有 Channel 持有 TCPConnection，是为了保证生命周期
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
	loop_->assertInLoopThread();
	int savedErrno = 0;
	ssize_t n = inputBuf_.readFd(socket_->fd(), &savedErrno);
	if (0 < n)
	{
		messageCallback_(shared_from_this(), &inputBuf_, receiveTime);
	}
	else if (0 == n)
	{
		LOG_WARN << "TcpConnection::handleRead() - [" << connName_
				 << "] - disconnected error: " << strerror_tl(savedErrno);
		handleClose();
	}
	else
	{
		errno = savedErrno;
		LOG_SYSERR << "TcpConnection::handleRead()";
		handleError();
	}
}

void TcpConnection::handleWrite()
{
	loop_->assertInLoopThread();
	if (channel_->isWriting())
	{
		const ssize_t nwrote = sockets::write(socket_->fd(),
											  outputBuf_.peek(),
											  outputBuf_.readableBytes());
		if (0 < nwrote)
		{
			outputBuf_.retrieve(nwrote);
			if (0 == outputBuf_.readableBytes())
			{
				channel_->disableWriting();
				if (writecompleteCallback_)
				{
					loop_->runInLoop(std::bind(writecompleteCallback_, shared_from_this()));
				}
				if (STATE::disconnecting == state_)	// 处理因 isWriting() 而暂停 shutdown 的 connection，确保所有数据全部发送出去
				{
					shutdownInLoop();
				}
			}
		}
		else
		{
			LOG_SYSERR << "TcpConnection::handelWrite() - error: " << strerror_tl(errno);
		}
	}
}

void TcpConnection::handleClose()
{
	loop_->assertInLoopThread();
	assert(STATE::connected == state_ || STATE::disconnecting == state_);
	setState(STATE::disconnected);
	channel_->disableAll();
	connectionCallback_(shared_from_this());

	closeCallback_(shared_from_this());
}

void TcpConnection::handleError()
{
	loop_->assertInLoopThread();	// 虽然没有必要，但是预防自己犯脑瘫
	int err = sockets::getSocketError(socket_->fd());
	LOG_ERROR << "TcpConnection::handleError() [" << connName_
			  << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}

#include "HttpServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpContext.h"
#include "TimingWheel.h"

#include "../net/TcpConnection.h"

#include "../net/EventLoop.h"
#include "../net/TimerId.h"
#include "../base/Logging.h"

#include <map>

namespace Misaka
{
namespace net
{
namespace http
{

	typedef TimingWheel::Entry Entry;
	typedef TimingWheel::EntryPtr EntryPtr;
	typedef TimingWheel::WeakEntryPtr WeakEntryPtr;
	typedef HttpResponse::HttpStatusCode HttpStatusCode;
	typedef HttpRequest::Version Version;

	struct TcpConnectionContext : copyable
	{
		TcpConnectionContext(const WeakEntryPtr& weakEntryPtr,
							const HttpContext& context) :
			weakEntryPtr_(weakEntryPtr),
			httpContext_(context)
		{ }

		WeakEntryPtr weakEntryPtr_;
		HttpContext httpContext_;
	};

}	// namespace http
}	// namespace net
}	// namespace Misaka

using namespace Misaka;
using namespace net;
using namespace net::http;

HttpServer::HttpServer(EventLoop* loop,
						const string& nameArg,
						const InetAddress& listenAddr,
						int idleTime) :
	timingWheelPtr_(CHECKNOTNULL(new TimingWheel(idleTime))),
	server_(loop,
			nameArg,
			listenAddr,
			TcpServer::Option::kReuseport)
{
	server_.setConnectionCallback(
		std::bind(&HttpServer::onConnection, this, _1));
	server_.setMessageCallback(
		std::bind(&HttpServer::onMessage, this, _1, _2, _3));
	loop->runEvery(1.0, std::bind(&TimingWheel::onTimer, timingWheelPtr_.get()));
}

void HttpServer::start()
{
	LOG_INFO << "HttpServer[" << server_.name()
		     << "] starts listening on " << server_.isPort();
	server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
	LOG_INFO << "HttpServer - " << conn->peerAddress().toIpPort() << " -> "
			 << conn->localAddress().toIpPort() << " is "
			 << (conn->connected() ? "UP" : "DOWN");
	
	if (conn->connected())
	{
		EntryPtr entryPtr(new Entry(conn));
		timingWheelPtr_->push_backEntryPtr(entryPtr);
		conn->setContext(std::move(TcpConnectionContext(
				WeakEntryPtr(entryPtr),
				HttpContext())
		));
	}
	// 这个回调注册，只有一个作用，初始化 http-conn 中的上下文
	// 即：清空内容，等待解析 url 的数据
}

void HttpServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime)
{
	LOG_DEBUG << conn->name() << " buffer -> "
			  << StringPiece(buf->peek(), static_cast<int>(buf->readableBytes()))
			  << " at " << receiveTime.toString();

	assert(conn->getContext().has_value());
	TcpConnectionContext* context = std::any_cast<TcpConnectionContext>(conn->getMutableContext());
	HttpContext* httpContext = &(context->httpContext_);
	
	if (!httpContext->parseRequest(buf, receiveTime))
	{
		conn->send(httpContext->request().versionString() + " 400 " +
					HttpResponse::message(HttpStatusCode::k400BadRequest));
		conn->shutdown();
	}

	if (httpContext->gotAll())	// 使用 if 语句，是为了处理 HTTP 请求包没有接收完整的情况，还需要继续接收
	{
		onRequest(conn, httpContext->request());
		httpContext->reset();

		// 放在 request 之后，减少响应时间对 connection 的生存时间产生影响
		// 因为 强制断开采用的是 shutdown，而非 forceclose，所以允许 write 事件处理完毕
		WeakEntryPtr weakEntryPtr = std::any_cast<TcpConnectionContext>(
			conn->getMutableContext())->weakEntryPtr_;
		EntryPtr entryPtr = weakEntryPtr.lock();
		if (timingWheelPtr_->containsInBackBucket(entryPtr))
			return;
		if (entryPtr)
		{
			timingWheelPtr_->push_backEntryPtr(std::move(entryPtr));
		}
		// 不需要对 else 进行处理
		// 触发 else 的情况，是在当前正处理 request 时，main-thread 已经检测到 xxx-conn 连接超时
		// 将其强行断开，因为使用 shutdown，所以会等待 request 完毕后，在销毁 TCPConnection
		// 这也就导致了 Entry 已经被销毁，而 TCPConnection 仍在存活
		// PS: else 的情况触发的可能性很低，因为有 handleWriting 事件的处理机制
	}

	// 这个回溯注册，分为两个部分：
	// 1. 解析 | 2. 响应
	// <1> 在 onMessage 完成
	// <2> 在 onRequest 完成 -> 只是将函数拆开，便于代码组织
}

void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req)
{
	const string& connection = req.getHeader("Connection");
	bool close = connection == "close" ||
		(req.version() == Version::kHttp10 && connection != "keep-alive");
	
	HttpResponse response(req.versionString(), close);
	httpCallback_(req, &response);
	Buffer buf;
	response.appendToBuffer(&buf);
	conn->send(&buf);
	if (response.closeConnection())
	{
		conn->shutdown();
	}
}
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
	// ����ص�ע�ᣬֻ��һ�����ã���ʼ�� http-conn �е�������
	// ����������ݣ��ȴ����� url ������
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

	if (httpContext->gotAll())	// ʹ�� if ��䣬��Ϊ�˴��� HTTP �����û�н������������������Ҫ��������
	{
		onRequest(conn, httpContext->request());
		httpContext->reset();

		// ���� request ֮�󣬼�����Ӧʱ��� connection ������ʱ�����Ӱ��
		// ��Ϊ ǿ�ƶϿ����õ��� shutdown������ forceclose���������� write �¼��������
		WeakEntryPtr weakEntryPtr = std::any_cast<TcpConnectionContext>(
			conn->getMutableContext())->weakEntryPtr_;
		EntryPtr entryPtr = weakEntryPtr.lock();
		if (timingWheelPtr_->containsInBackBucket(entryPtr))
			return;
		if (entryPtr)
		{
			timingWheelPtr_->push_backEntryPtr(std::move(entryPtr));
		}
		// ����Ҫ�� else ���д���
		// ���� else ����������ڵ�ǰ������ request ʱ��main-thread �Ѿ���⵽ xxx-conn ���ӳ�ʱ
		// ����ǿ�жϿ�����Ϊʹ�� shutdown�����Ի�ȴ� request ��Ϻ������� TCPConnection
		// ��Ҳ�͵����� Entry �Ѿ������٣��� TCPConnection ���ڴ��
		// PS: else ����������Ŀ����Ժܵͣ���Ϊ�� handleWriting �¼��Ĵ������
	}

	// �������ע�ᣬ��Ϊ�������֣�
	// 1. ���� | 2. ��Ӧ
	// <1> �� onMessage ���
	// <2> �� onRequest ��� -> ֻ�ǽ������𿪣����ڴ�����֯
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
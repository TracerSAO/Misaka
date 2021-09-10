#ifndef MISAKA_HTTPSERVER_H
#define MISAKA_HTTPSERVER_H

#include "../base/copyable.h"
#include "../base/Types.h"

#include "../net/TcpServer.h"

namespace Misaka
{
namespace net
{

class EventLoop;
class InetAddress;

namespace http
{

class HttpRequest;
class HttpResponse;
class TimingWheel;

class HttpServer : noncopyable
{
public:
	typedef std::function<void (const HttpRequest&, HttpResponse*)> HttpCallback;
public:
	HttpServer(EventLoop* loop,
				const string& nameArg,
				const InetAddress& listenAddr,
				int idleTime = 10);

	void start();

	void setThreadNum(int numThreads)
	{
		server_.setThreadNum(numThreads);
	}

	void setHttpCallback(HttpCallback cb)
	{
		httpCallback_ = cb;
	}

private:
	void onConnection(const TcpConnectionPtr&);
	void onMessage(const TcpConnectionPtr&, Buffer*, Timestamp);
	void onRequest(const TcpConnectionPtr&, const HttpRequest&);

private:
	HttpCallback httpCallback_;
	std::shared_ptr<TimingWheel> timingWheelPtr_;
	TcpServer server_;
};

}	// namespace http
}	// namespace net
}	// namespace Misaka

#endif // !MISAKA_HTTPSERVER_H

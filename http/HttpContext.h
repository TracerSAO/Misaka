#ifndef MISAKA_HTTPCONTEXT_H
#define MISAKA_HTTPCONTEXT_H

#include "HttpRequest.h"

namespace Misaka
{

class Timestamp;

namespace net
{

class Buffer;

namespace http
{

class HttpContext : copyable
{
public:
	enum class HttpRequestParseState
	{
		kExpectRequestLine,
		kExpectHeads,
		kExpectBody,
		kGotAll
	};
public:
	HttpContext() :
		state_(HttpRequestParseState::kExpectRequestLine)
	{
	}

	bool parseRequest(Buffer*, Timestamp);

	bool gotAll() const
	{ return HttpRequestParseState::kGotAll == state_; }

	void reset()
	{
		state_ = HttpRequestParseState::kExpectRequestLine;
		HttpRequest temp;
		req_.swap(temp);
	}

	HttpRequest& request()
	{ return req_; }

	const HttpRequest& request() const
	{ return req_; }

private:
	bool parseReuestLine(const char* begin, const char* end);
	
	static const char* nextUnSpaceIndex(const char* index, const char* end);

private:
	HttpRequestParseState state_;
	HttpRequest req_;
};

}	// namespace http
}	// namespace net
}	// namespace Misaka

#endif // !MISAKA_HTTPCONTEXT_H
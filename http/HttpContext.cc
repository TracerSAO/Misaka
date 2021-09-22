#include "../net/Buffer.h"
#include "../base/Timestamp.h"
#include "../base/Logging.h"
#include "HttpContext.h"

#include <algorithm>

using namespace Misaka;
using namespace net;
using namespace net::http;

const char* HttpContext::nextUnSpaceIndex(const char* index, const char* end)
{
	while (index != end && ' ' == *index)
		index++;
	return index;
}

bool HttpContext::parseReuestLine(const char* begin, const char* end)
{
	assert(HttpRequestParseState::kExpectRequestLine == state_);
	// method
	begin = nextUnSpaceIndex(begin, end);
	const char* index = std::find(begin, end, ' ');
	if (index == end)
		return false;
	req_.setMethod(begin, index);

	// path	-> a/b/c?dddd http/1.1
	begin = nextUnSpaceIndex(index, end);
	index = std::find(begin, end, '?');
	if (index != end)
	{
		req_.setPath(begin, index);
		begin = index + 1;
		index = std::find(index + 1, end, ' ');
		req_.setQuery(begin, index);
	}
	else
	{
		index = std::find(begin, end, ' ');
		req_.setPath(begin, index);
	}

	// version
	begin = nextUnSpaceIndex(index, end);
	HttpRequest::Version ver = HttpRequest::Version::kUnKnown;
	string verStr(begin, end);
	if ("HTTP/1.0" == verStr)
		ver = HttpRequest::Version::kHttp10;
	else if ("HTTP/1.1" == verStr)
		ver = HttpRequest::Version::kHttp11;
	req_.setVersion(ver);

	return HttpRequest::Version::kUnKnown != ver ? true : false;
}

bool HttpContext::parseRequest(Buffer* buf, Timestamp receiveTime)
{
	bool ok = true;
	bool hasMore = true;

	while (hasMore)
	{
		if (HttpRequestParseState::kExpectRequestLine == state_)
		{
			const char* crlf = buf->findCRLF();
			if (crlf)
			{
				ok = parseReuestLine(buf->peek(), crlf);
				if (ok)
				{
					req_.setReceiveTime(receiveTime);
					buf->retrieveUntil(crlf + 2);
					state_ = HttpRequestParseState::kExpectHeads;
					LOG_TRACE << "RequestLine -> Heads";
				}
				else
				{
					hasMore = false;
				}
			}
			else
			{
				hasMore = false;	// 本次解析的数据包，不够组成一个完成的 HTTP 请求包，等待下次再继续解析
			}
		}
		else if (HttpRequestParseState::kExpectHeads == state_)
		{
			const char* crlf = buf->findCRLF();
			if (crlf)
			{
				const char* colon = std::find(buf->peek(), crlf, ':');
				if (colon != crlf)
				{
					req_.addHeader(buf->peek(), colon, crlf);
				}
				else
				{
					// <A: B>[CRLF][CRLF] -> end of heads , empty line
					HttpRequest::Method m = req_.method();
					if (HttpRequest::Method::kGet != m)
					{
						state_ = HttpRequestParseState::kExpectBody;
						LOG_TRACE << "Heads -> Body";
					}
					else
					{
						state_ = HttpRequestParseState::kGotAll;
						hasMore = false;
						LOG_TRACE << "Heads -> GotAll";
					}
				}
				buf->retrieveUntil(crlf + 2);
			}
			else
			{
				hasMore = false;
			}
		}
		else
		{
			string contentLengthStr = req_.getHeader("Content-Length");
			if (!contentLengthStr.empty())
			{
				size_t size = buf->readableBytes();
				req_.setBody(buf->peek(), buf->beginWrite());
				buf->retrieveAll();
				hasMore = false;
				if (static_cast<int>(size) >= atoi(contentLengthStr.c_str()))
				{
					state_ = HttpRequestParseState::kGotAll;
					hasMore = false;
					LOG_TRACE << "Body -> GotAll";
				}
			}
			else
			{
				state_ = HttpRequestParseState::kGotAll;
				hasMore = false;
				LOG_TRACE << "Body -> GotAll";
			}
		}
	}
	return ok;
}
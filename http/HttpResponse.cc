#include "../net/Buffer.h"
#include "HttpResponse.h"

using namespace Misaka;
using namespace net;
using namespace net::http;

std::map<HttpResponse::HttpStatusCode, const char*>
HttpResponse::statusMessageCollect_
{
	{ HttpResponse::HttpStatusCode::k200OK, "OK" },
	{ HttpResponse::HttpStatusCode::k400BadRequest, "Bad Request" },
	{ HttpResponse::HttpStatusCode::k403Forbidden, "Forbidden" },
	{ HttpResponse::HttpStatusCode::k404NotFound, "Not Found" },
	{ HttpResponse::HttpStatusCode::k500InternalError, "Internal Error" },
	{ HttpResponse::HttpStatusCode::kUnknown, "Unknow status" }
};

void HttpResponse::appendToBuffer(Buffer* buffer) const
{
	// add Response-Line
	char buf[32];
	snprintf(buf, sizeof buf, "%s %d ", version_.c_str(), static_cast<int>(statuCode_));
	buffer->append(buf);								// -> StringPiece() -> char*, int
	buffer->append(statusMessageCollect_[statuCode_]);	// -> StringPiece() -> char*, int
	buffer->append("\r\n");

	// add Response-Head
	if (!closeConnection())	// Connection:: Keep-Alive
	{
		snprintf(buf, sizeof buf, "Content-Length: %d\r\n", static_cast<int>(body_.size()));
		buffer->append(buf);
		buffer->append("Connection: keep-alive\r\n");
	}
	else	// Connection: close
	{
		buffer->append("Connection: close\r\n");
	}

	for (const auto& item : headers_)
	{
		buffer->append(item.first);
		buffer->append(": ");
		buffer->append(item.second);
		buffer->append("\r\n");
	}

	// add Response-Body
	buffer->append("\r\n");
	buffer->append(body_);
}

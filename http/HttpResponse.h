#ifndef MISAKA_HTTPRESPONSE_H
#define MISAKA_HTTPRESPONSE_H

#include "../base/copyable.h"
#include "../base/Types.h"
#include "../base/StringPiece.h"

#include <map>

namespace Misaka
{
namespace net
{

class Buffer;

namespace http
{

class HttpResponse : copyable
{
public:
	enum class HttpStatusCode : int
	{
		kUnknown,
		k200OK = 200,
		k400BadRequest = 400,
		k403Forbidden = 403,
		k404NotFound = 404,
		k500InternalError = 500
	};
public:
	HttpResponse(const string& version, bool close) :
		version_(version),
		closeconnection_(close)
	{
	}

	void setStatusCode(HttpStatusCode code)
	{ statusCode_ = code; }

	// void setStatusMessage(const string& meg)
	// { statuMessage_ = meg;}

	static const char* message(HttpStatusCode code)
	{ return statusMessageCollect_[code]; }

	bool closeConnection() const
	{ return closeconnection_; }

	void setCloseConnection(bool close)
	{ closeconnection_ = close; }

	void setContentType(const string& contentType)
	{ headers_["Content-Type"] = contentType; }

	void addHeader(const string& field, const string& val)
	{ headers_[field] = val; }

	void setBody(const string& body)
	{ body_ = body; }

	void setBody(const char* buf, size_t len)
	{ body_.assign(buf, len); }

	void appendToBuffer(Buffer*) const;

private:
	HttpStatusCode statusCode_;
	string version_;
	// string statuMessage_;
	bool closeconnection_;
	std::map<string, string> headers_;
	std::string body_;

	static std::map<HttpStatusCode, const char*> statusMessageCollect_;
};

}	// namespace http
}	// namespace net
}	// namespace Misaka

#endif // !MISAKA_HTTPRESPONSE_H

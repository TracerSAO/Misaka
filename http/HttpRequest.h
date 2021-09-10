#ifndef MISAKA_HTTPREQUEST_H
#define MISAKA_HTTPREQUEST_H

#include "../base/copyable.h"
#include "../base/Types.h"
#include "../base/Timestamp.h"

#include <cassert>
#include <cstdlib>

#include <map>

namespace Misaka
{
namespace net
{
namespace http
{

class HttpRequest : copyable
{
public:
	enum class Method
	{
		kInvalid, kGet, kPost, kHead, kPut, kDelete
	};

	enum class Version
	{
		kUnKnown, kHttp10, kHttp11
	};

public:
	HttpRequest() :
		method_(Method::kInvalid),
		version_(Version::kUnKnown)
	{
	}
	
	bool setMethod(const char* begin, const char* end)
	{
		assert(Method::kInvalid == method_);
		string m(begin, end);
		if ("GET" == m)
		{
			method_ = Method::kGet;
		}
		else if ("POST" == m)
		{
			method_ = Method::kPost;
		}
		else if ("HEAD" == m)
		{
			method_ = Method::kHead;
		}
		else if ("PUT" == m)
		{
			method_ = Method::kPut;
		}
		else if ("DELETE" == m)
		{
			method_ = Method::kDelete;
		}
		else
		{
			method_ = Method::kInvalid;
		}
		return Method::kInvalid != method_;
	}
		
	Method method() const
	{ return method_; }
		
	string methodString() const
	{
		const char* res = nullptr;
		switch (method_)
		{
		case Method::kGet:
			res = "GET";
			break;
		case Method::kHead:
			res = "HEAD";
			break;
		case Method::kPost:
			res = "POST";
			break;
		case Method::kPut:
			res = "PUT";
			break;
		case Method::kDelete:
			res = "DELETE";
			break;
		default:
			res = "unkonwn-method";
			break;
		}
		return res;
	}

	void setPath(const char* begin, const char* end)
	{ path_.assign(begin, end); }
		
	const string& path() const
	{ return path_; }

	void setQuery(const char* begin, const char* end)
	{ query_.assign(begin, end); }

	void setVersion(Version v)
	{ version_ = v; }
		
	Version version() const
	{ return version_; }

	string versionString() const
	{
		switch (version())
		{
		case HttpRequest::Version::kHttp10:
			return "HTTP/1.0";
		case HttpRequest::Version::kHttp11:
			return "HTTP/1.1";
		default:
			return "unknown-Version";
		}
	}

	void setReceiveTime(Timestamp t)
	{ receiveTime_ = t; }
		
	Timestamp receiveTime() const
	{ return receiveTime_; }

	void addHeader(const char* begin, const char* colon, const char* end)
	{
		string field(begin, colon);
		colon++;
		while (end != colon && ' ' == *colon)	// 清理多余的 ' '
			colon++;
		string val(colon, end);
		while (!val.empty() && ' ' == val[val.size() - 1])	// 清理后方 ' '
			val.resize(val.size() - 1);
		headers_[field] = string(colon, end);
	}

	string getHeader(const string& field) const
	{
		string res;
		if (headers_.find(field) != headers_.end())
			res = headers_.at(field);
		return res;
	}
		
	const std::map<string, string>& headers() const
	{ return headers_; }

	void setBody(const char* begin, const char* end)
	{ body_.assign(begin, end); }

	const string& body() const
	{ return body_; }

	void swap(HttpRequest& req)
	{
		std::swap(method_, req.method_);
		std::swap(version_, req.version_);
		path_.swap(req.path_);
		query_.swap(req.query_);
		receiveTime_.swap(req.receiveTime_);
		headers_.swap(req.headers_);
		body_.swap(req.body_);
	}

private:
	Method method_;
	string path_;
	string query_;
	Version version_;
	Timestamp receiveTime_;
	std::map<string, string> headers_;
	string body_;
};

}	// namespace http
}	// namespace net
}	// namespace Misaka

#endif // !MISAKA_HTTPREQUEST_H
